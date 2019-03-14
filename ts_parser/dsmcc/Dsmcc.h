#ifndef _DSMCC_H_
#define _DSMCC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsAribCommon.h"
#include "SectionParser.h"
#include "DsmccDescriptorDefs.h"


class CDsmccAdaptationHeader
{
public:
	CDsmccAdaptationHeader (void)
		:adaptationType (0)
	{
		memset (adaptationDataByte, 0x00, sizeof(adaptationDataByte));
	}
	virtual ~CDsmccAdaptationHeader (void) {}


	uint8_t adaptationType;
	uint8_t adaptationDataByte [0xff];
};

class CDsmccMessageHeader
{
public:
	CDsmccMessageHeader (void)
		:protocolDiscriminator (0)
		,dsmccType (0)
		,messageId (0)
		,transaction_id (0)
		,reserved (0)
		,adaptationLength (0)
		,messageLength (0)
	{}
	virtual ~CDsmccMessageHeader (void) {}


	uint8_t protocolDiscriminator;
	uint8_t dsmccType;
	uint16_t messageId;
	uint32_t transaction_id;
	uint8_t reserved;
	uint8_t adaptationLength;
	uint16_t messageLength;
	CDsmccAdaptationHeader dsmccAdaptationHeader;
};

// DII
class CDownloadInfoIndication
{
public:
	class CModule {
	public:
		CModule (void)
			:moduleId (0)
			,moduleSize (0)
			,moduleVersion (0)
			,moduleInfoLength (0)
		{
			memset (moduleInfoByte, 0x00, sizeof(moduleInfoByte));
		}
		virtual ~CModule (void) {}

		uint16_t moduleId;
		uint32_t moduleSize;
		uint8_t moduleVersion;
		uint8_t moduleInfoLength;
		uint8_t moduleInfoByte [0xff];
	};

public:
	CDownloadInfoIndication (void)
		:downloadId (0)
		,blockSize (0)
		,windowSize (0)
		,ackPeriod (0)
		,tCDownloadWindow (0)
		,tCDownloadScenario (0)
		,numberOfModules (0)
		,privateDataLength (0)
	{
		modules.clear();
		memset (privateDataByte, 0x00, sizeof(privateDataByte));
	}
	virtual ~CDownloadInfoIndication (void)
	{
		modules.clear();
	}

	
	CDsmccMessageHeader dsmccMessageHeader;
	uint32_t downloadId;
	uint16_t blockSize;
	uint8_t windowSize;
	uint8_t ackPeriod;
	uint32_t tCDownloadWindow;
	uint32_t tCDownloadScenario;
	// compatibilityDescriptor() // 4bytes?
	uint16_t numberOfModules;
	std::vector <CModule> modules;
	uint16_t privateDataLength;
	uint8_t privateDataByte [0xffff];

};

class CDsmccDownloadDataHeader
{
public:
	CDsmccDownloadDataHeader (void)
		:protocolDiscriminator (0)
		,dsmccType (0)
		,messageId (0)
		,downloadId (0)
		,reserved (0)
		,adaptationLength (0)
		,messageLength (0)
	{}
	virtual ~CDsmccDownloadDataHeader (void) {}

	uint8_t protocolDiscriminator;
	uint8_t dsmccType;
	uint16_t messageId;
	uint32_t downloadId;
	uint8_t reserved;
	uint8_t adaptationLength;
	uint16_t messageLength;
	CDsmccAdaptationHeader dsmccAdaptationHeader;
};

#define DDB_FIX_LEN		(6) // moduleId      16  uimsbf
							// moduleVersion  8  uimsbf
							// reserved       8  uimsbf
							// blockNumber   16  uimsbf

// DDB
class CDownloadDataBlock
{
public:
	CDownloadDataBlock (void)
		:moduleId (0)
		,moduleVersion (0)
		,reserved (0)
		,blockNumber (0)
		,m_blockDataByteLen (0)
	{
		memset (blockDataByte, 0x00, sizeof(blockDataByte));
	}
	virtual ~CDownloadDataBlock (void) {}

	CDsmccDownloadDataHeader dsmccDownloadDataHeader;
	uint16_t moduleId;
	uint8_t moduleVersion;
	uint8_t reserved;
	uint16_t blockNumber;
	uint8_t blockDataByte [0xffff];

	int m_blockDataByteLen; // for blockDataByte
};


class CDsmcc : public CSectionParser
{
public:
	class CDII {
	public:
		CDII (void) {}
		virtual ~CDII (void) {}

		ST_SECTION_HEADER header;
		CDownloadInfoIndication dii;
	};

	class CDDB {
	public:
		CDDB (void) {}
		virtual ~CDDB (void) {}

		ST_SECTION_HEADER header;
		CDownloadDataBlock ddb;
	};

public:
	explicit CDsmcc (size_t poolSize);
	virtual ~CDsmcc (void);

	bool onSectionStarted (const CSectionInfo *pSection) override;
	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpDIIs (void) const;
	void dumpDDBs (void) const;

	void dumpDII (const CDII* pDII) const;
	void dumpDDB (const CDDB* pDDB) const;

	void clear (void);


private:
	bool parseDII (const CSectionInfo *pCompSection, CDII* pOutDII);
	bool parseDDB (const CSectionInfo *pCompSection, CDDB* pOutDDB);

	void releaseDIIs (void);
	void releaseDDBs (void);

	std::vector <CDII*> mDIIs;
	std::vector <CDDB*> mDDBs;
};

#endif
