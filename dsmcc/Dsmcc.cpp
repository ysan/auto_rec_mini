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

	pDII->dii.messageHeader.protocolDiscriminator = *p;
	p += 1;
	pDII->dii.messageHeader.dsmccType = *p;
	p += 1;
	pDII->dii.messageHeader.messageId = *p << 8 | *(p+1);
	p += 2;
	pDII->dii.messageHeader.transaction_id = *p << 24 | *(p+1) << 16 | *(p+2) << 8 | *(p+3);
	p += 4;
	pDII->dii.messageHeader.reserved = *p;
	p += 1;
	pDII->dii.messageHeader.adaptationLength = *p;
	p += 1;
	pDII->dii.messageHeader.messageLength = *p << 8 | *(p+1);
	p += 2;
	int adaptLen = pDII->dii.messageHeader.adaptationLength;
	if (adaptLen > 0) {
		pDII->dii.messageHeader.dsmccAdaptationHeader.adaptationType = *p;
		memcpy (pDII->dii.messageHeader.dsmccAdaptationHeader.adaptationDataByte, p+1, adaptLen -1);
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
	memcpy (pDII->dii.privateDataByte, p, pDII->dii.privateDataLength);
	p += (int)pDII->dii.privateDataLength;

	// DII length check
	int len = p - pCompSection->getDataPartAddr();
	if (len != pCompSection->getDataPartLen()) {
		_UTL_LOG_W ("dsmcc: DII length is invalid... [%d:%d]", len, pCompSection->getDataPartLen());
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

void CDsmcc::dumpDII (const CDII* pDII) const
{
	if (!pDII) {
		return;
	}
	
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("========================================\n");
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

void CDsmcc::dumpDDB (const CDDB* pDDB) const
{
	if (!pDDB) {
		return;
	}
	
	_UTL_LOG_I ("========================================\n");

	_UTL_LOG_I ("========================================\n");
}

void CDsmcc::clear (void)
{
	releaseDDBs ();
	releaseDIIs ();
	detachAllSectionList ();
}
