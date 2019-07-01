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
	mDIIs.clear();
	mDDBs.clear();
}

CDsmcc::~CDsmcc (void)
{
	clear();
}


bool CDsmcc::onSectionStarted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return false;
	}

	ST_SECTION_HEADER const *pHdr = const_cast<CSectionInfo*>(pCompSection)->getHeader();
	if (!pHdr) {
		return false;
	}

	if ((pHdr->table_id == 0x3b) || (pHdr->table_id == 0x3c)) {
		return true;
	}

	_UTL_LOG_W ("dsmcc: unsupported table_id:[0x%02x]", pHdr->table_id);
	return false;
}

void CDsmcc::onSectionCompleted (const CSectionInfo *pCompSection)
{
	if (!pCompSection) {
		return ;
	}

	ST_SECTION_HEADER const *pHdr = const_cast<CSectionInfo*>(pCompSection)->getHeader();
	if (!pHdr) {
		return ;
	}

	if (pHdr->table_id == 0x3b) {

		CDII *pDII = new CDII ();
		if (!parseDII (pCompSection, pDII)) {
			delete pDII;
			pDII = NULL;
			detachSectionList (pCompSection);
			return ;
		}

		mDIIs.push_back (pDII);
		dumpDII (pDII);

//TODO
//detachSectionList (pCompSection);

	} else if (pHdr->table_id == 0x3c) {

		CDDB *pDDB = new CDDB ();
		if (!parseDDB (pCompSection, pDDB)) {
			delete pDDB;
			pDDB = NULL;
			detachSectionList (pCompSection);
			return ;
		}

		mDDBs.push_back (pDDB);
		dumpDDB (pDDB);

//TODO
//detachSectionList (pCompSection);

	} else {
		_UTL_LOG_W ("dsmcc: unexpected table_id:[0x%02x]", pHdr->table_id);
		detachSectionList (pCompSection);
	}
}

bool CDsmcc::parseDII (const CSectionInfo *pCompSection, CDII* pOutDII)
{
	if (!pCompSection || !pOutDII) {
		return false;
	}

	uint8_t *p = NULL; // work
	CDII* pDII = pOutDII;

	pDII->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();

	pDII->dii.dsmccMessageHeader.protocolDiscriminator = *p;
	p += 1;
	pDII->dii.dsmccMessageHeader.dsmccType = *p;
	p += 1;
	pDII->dii.dsmccMessageHeader.messageId = *p << 8 | *(p+1);
	p += 2;
	pDII->dii.dsmccMessageHeader.transaction_id = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	pDII->dii.dsmccMessageHeader.reserved = *p;
	p += 1;
	pDII->dii.dsmccMessageHeader.adaptationLength = *p;
	p += 1;
	pDII->dii.dsmccMessageHeader.messageLength = *p << 8 | *(p+1);
	p += 2;
	int adaptLen = pDII->dii.dsmccMessageHeader.adaptationLength;
	if (adaptLen > 0) {
		pDII->dii.dsmccMessageHeader.dsmccAdaptationHeader.adaptationType = *p;
		memcpy (pDII->dii.dsmccMessageHeader.dsmccAdaptationHeader.adaptationDataByte, p+1, adaptLen -1);
		p += adaptLen;
	}

	pDII->dii.downloadId = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	pDII->dii.blockSize = *p << 8 | *(p+1);
	p += 2;
	pDII->dii.windowSize = *p;
	p += 1;
	pDII->dii.ackPeriod = *p;
	p += 1;
	pDII->dii.tCDownloadWindow = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	pDII->dii.tCDownloadScenario = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;

	// compatibilityDescriptor() // 4bytes?
	uint16_t compatibilityDescriptorLength = *p << 8 | *(p+1);
	int cdLen = compatibilityDescriptorLength;
	if (cdLen > 0) {
		p += 2 + cdLen;
	} else {
		p += 4;
	}

	pDII->dii.numberOfModules = *p << 8 | *(p+1);
	p += 2;
	int moduleNum = pDII->dii.numberOfModules;
	while (moduleNum > 0) {
		CDownloadInfoIndication::CModule mod;
		mod.moduleId = *p << 8 | *(p+1);
		p += 2;
		mod.moduleSize = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
		p += 4;
		mod.moduleVersion = *p;
		p += 1;
		mod.moduleInfoLength = *p;
		p += 1;
		memcpy (mod.moduleInfoByte, p, mod.moduleInfoLength);
		p += (int)mod.moduleInfoLength;
		pDII->dii.modules.push_back (mod);
		-- moduleNum;
	}
	
	pDII->dii.privateDataLength = *p << 8 | *(p+1);
	p += 2;
	memcpy (pDII->dii.privateDataByte, p, (int)pDII->dii.privateDataLength);
	p += (int)pDII->dii.privateDataLength;

	// DII length check
	int totalLen = p - pCompSection->getDataPartAddr();
	if (totalLen != pCompSection->getDataPartLen()) {
		_UTL_LOG_W ("dsmcc: DII length is invalid... [%d:%d]", totalLen, pCompSection->getDataPartLen());
		return false;
	}

	return true;
}

bool CDsmcc::parseDDB (const CSectionInfo *pCompSection, CDDB* pOutDDB)
{
	if (!pCompSection || !pOutDDB) {
		return false;
	}

	uint8_t *p = NULL; // work
	CDDB* pDDB = pOutDDB;

	pDDB->header = *(const_cast<CSectionInfo*>(pCompSection)->getHeader());

	p = pCompSection->getDataPartAddr();

	pDDB->ddb.dsmccDownloadDataHeader.protocolDiscriminator = *p;
	p += 1;
	pDDB->ddb.dsmccDownloadDataHeader.dsmccType = *p;
	p += 1;
	pDDB->ddb.dsmccDownloadDataHeader.messageId = *p << 8 | *(p+1);
	p += 2;
	pDDB->ddb.dsmccDownloadDataHeader.downloadId = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	pDDB->ddb.dsmccDownloadDataHeader.reserved = *p;
	p += 1;
	pDDB->ddb.dsmccDownloadDataHeader.adaptationLength = *p;
	p += 1;
	pDDB->ddb.dsmccDownloadDataHeader.messageLength = *p << 8 | *(p+1);
	p += 2;
	int adaptLen = pDDB->ddb.dsmccDownloadDataHeader.adaptationLength;
	if (adaptLen > 0) {
		pDDB->ddb.dsmccDownloadDataHeader.dsmccAdaptationHeader.adaptationType = *p;
		memcpy (pDDB->ddb.dsmccDownloadDataHeader.dsmccAdaptationHeader.adaptationDataByte, p+1, adaptLen -1);
		p += adaptLen;
	}

	pDDB->ddb.moduleId = *p << 8 | *(p+1);
	p += 2;
	pDDB->ddb.moduleVersion = *p;
	p += 1;
	pDDB->ddb.reserved = *p;
	p += 1;
	pDDB->ddb.blockNumber = *p << 8 | *(p+1);
	p += 2;
	pDDB->ddb.m_blockDataByteLen = pDDB->ddb.dsmccDownloadDataHeader.messageLength - adaptLen - DDB_FIX_LEN;
	if (pDDB->ddb.m_blockDataByteLen < 0) {
		_UTL_LOG_W ("dsmcc: m_blockDataByteLen invalid [%d]", pDDB->ddb.m_blockDataByteLen);
		return false;
	}
	memcpy (pDDB->ddb.blockDataByte, p, pDDB->ddb.m_blockDataByteLen);
	p += pDDB->ddb.m_blockDataByteLen;

	// DDB length check
	int totalLen = p - pCompSection->getDataPartAddr();
	if (totalLen != pCompSection->getDataPartLen()) {
		_UTL_LOG_W ("dsmcc: DDB length is invalid... [%d:%d]", totalLen, pCompSection->getDataPartLen());
		return false;
	}

	return true;
}

void CDsmcc::releaseDIIs (void)
{
	if (mDIIs.size() == 0) {
		return;
	}

	std::vector<CDII*>::iterator iter = mDIIs.begin(); 
	for (; iter != mDIIs.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mDIIs.clear();
}

void CDsmcc::releaseDDBs (void)
{
	if (mDDBs.size() == 0) {
		return;
	}

	std::vector<CDDB*>::iterator iter = mDDBs.begin(); 
	for (; iter != mDDBs.end(); ++ iter) {
		delete (*iter);
		(*iter) = NULL;
	}

	mDDBs.clear();
}

void CDsmcc::dumpDIIs (void) const
{
	if (mDIIs.size() == 0) {
		return;
	}

	std::vector<CDII*>::const_iterator iter = mDIIs.begin(); 
	for (; iter != mDIIs.end(); ++ iter) {
		CDII *pDII = *iter;
		dumpDII (pDII);
	}
}

void CDsmcc::dumpDDBs (void) const
{
	if (mDDBs.size() == 0) {
		return;
	}

	std::vector<CDDB*>::const_iterator iter = mDDBs.begin(); 
	for (; iter != mDDBs.end(); ++ iter) {
		CDDB *pDDB = *iter;
		dumpDDB (pDDB);
	}
}

void CDsmcc::dumpDII (const CDII* pDII) const
{
	if (!pDII) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id           [0x%02x]\n", pDII->header.table_id);

	_UTL_LOG_I ("dsmccMessageHeader.protocolDiscriminator                [0x%02x]\n", pDII->dii.dsmccMessageHeader.protocolDiscriminator);
	_UTL_LOG_I ("dsmccMessageHeader.dsmccType                            [0x%02x]\n", pDII->dii.dsmccMessageHeader.dsmccType);
	_UTL_LOG_I ("dsmccMessageHeader.messageId                            [0x%04x]\n", pDII->dii.dsmccMessageHeader.messageId);
	_UTL_LOG_I ("dsmccMessageHeader.transaction_id                       [0x%08x]\n", pDII->dii.dsmccMessageHeader.transaction_id);
	_UTL_LOG_I ("dsmccMessageHeader.adaptationLength                     [%d]\n", pDII->dii.dsmccMessageHeader.adaptationLength);
	_UTL_LOG_I ("dsmccMessageHeader.messageLength                        [%d]\n", pDII->dii.dsmccMessageHeader.messageLength);
	int adaptLen = pDII->dii.dsmccMessageHeader.adaptationLength;
	if (adaptLen > 0) {
		_UTL_LOG_I ("dsmccMessageHeader.dsmccAdaptationHeader.adaptationType [0x%02x]\n", pDII->dii.dsmccMessageHeader.dsmccAdaptationHeader.adaptationType);
		CUtils::dumper (pDII->dii.dsmccMessageHeader.dsmccAdaptationHeader.adaptationDataByte, pDII->dii.dsmccMessageHeader.adaptationLength);
	}

	_UTL_LOG_I ("downloadId         [0x%08x]\n", pDII->dii.downloadId);
	_UTL_LOG_I ("blockSize          [%d]\n", pDII->dii.blockSize);
	_UTL_LOG_I ("windowSize         [%d]\n", pDII->dii.windowSize);
	_UTL_LOG_I ("ackPeriod          [0x%02x]\n", pDII->dii.ackPeriod);
	_UTL_LOG_I ("tCDownloadWindow   [0x%08x]\n", pDII->dii.tCDownloadWindow);
	_UTL_LOG_I ("tCDownloadScenario [0x%08x]\n", pDII->dii.tCDownloadScenario);
	_UTL_LOG_I ("numberOfModules    [%d]\n", pDII->dii.numberOfModules);

	std::vector<CDownloadInfoIndication::CModule>::const_iterator iter_mod = pDII->dii.modules.begin();
	for (; iter_mod != pDII->dii.modules.end(); ++ iter_mod) {
		_UTL_LOG_I ("\n--  module  --\n");
        _UTL_LOG_I ("moduleId         [0x%04x]\n", iter_mod->moduleId);
        _UTL_LOG_I ("moduleSize       [%d]\n", iter_mod->moduleSize);
        _UTL_LOG_I ("moduleVersion    [0x%02x]\n", iter_mod->moduleVersion);
        _UTL_LOG_I ("moduleInfoLength [%d]\n", iter_mod->moduleInfoLength);
		CUtils::dumper (iter_mod->moduleInfoByte, iter_mod->moduleInfoLength);
	}

	_UTL_LOG_I ("privateDataLength  [%d]\n", pDII->dii.privateDataLength);
	if ((int)pDII->dii.privateDataLength > 0) {
		CUtils::dumper (pDII->dii.privateDataByte, pDII->dii.privateDataLength);
	}

	_UTL_LOG_I ("\n");
}

void CDsmcc::dumpDDB (const CDDB* pDDB) const
{
	if (!pDDB) {
		return;
	}

	_UTL_LOG_I (__PRETTY_FUNCTION__);
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("table_id         [0x%02x]\n", pDDB->header.table_id);

	_UTL_LOG_I ("dsmccDownloadDataHeader.protocolDiscriminator                [0x%02x]\n", pDDB->ddb.dsmccDownloadDataHeader.protocolDiscriminator);
	_UTL_LOG_I ("dsmccDownloadDataHeader.dsmccType                            [0x%02x]\n", pDDB->ddb.dsmccDownloadDataHeader.dsmccType);
	_UTL_LOG_I ("dsmccDownloadDataHeader.messageId                            [0x%04x]\n", pDDB->ddb.dsmccDownloadDataHeader.messageId);
	_UTL_LOG_I ("dsmccDownloadDataHeader.downloadId                           [0x%08x]\n", pDDB->ddb.dsmccDownloadDataHeader.downloadId);
	_UTL_LOG_I ("dsmccDownloadDataHeader.adaptationLength                     [%d]\n", pDDB->ddb.dsmccDownloadDataHeader.adaptationLength);
	_UTL_LOG_I ("dsmccDownloadDataHeader.messageLength                        [%d]\n", pDDB->ddb.dsmccDownloadDataHeader.messageLength);
	int adaptLen = pDDB->ddb.dsmccDownloadDataHeader.adaptationLength;
	if (adaptLen > 0) {
		_UTL_LOG_I ("dsmccDownloadDataHeader.dsmccAdaptationHeader.adaptationType [0x%02x]\n", pDDB->ddb.dsmccDownloadDataHeader.dsmccAdaptationHeader.adaptationType);
		CUtils::dumper (pDDB->ddb.dsmccDownloadDataHeader.dsmccAdaptationHeader.adaptationDataByte, pDDB->ddb.dsmccDownloadDataHeader.adaptationLength);
	}

	_UTL_LOG_I ("moduleId         [0x%04x]\n", pDDB->ddb.moduleId);
	_UTL_LOG_I ("moduleVersion    [0x%02x]\n", pDDB->ddb.moduleVersion);
	_UTL_LOG_I ("blockNumber      [%d]\n", pDDB->ddb.blockNumber);
	if (pDDB->ddb.m_blockDataByteLen > 0) {
		CUtils::dumper (pDDB->ddb.blockDataByte, pDDB->ddb.m_blockDataByteLen);
	}

	_UTL_LOG_I ("\n");
}

void CDsmcc::clear (void)
{
	releaseDDBs ();
	releaseDIIs ();
	detachAllSectionList ();
}
