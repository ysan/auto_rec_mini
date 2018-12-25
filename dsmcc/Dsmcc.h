#ifndef _DSMCC_H_
#define _DSMCC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <vector>

#include "Defs.h"
#include "TsCommon.h"
#include "SectionParser.h"
#include "DescriptorDefs.h"


//#define BIT_FIX_LEN				(2) // reserved_future_use        3  bslbf
									// broadcast_view_propriety   1  uimsbf
									// first_descriptors_length  12  uimsbf

//#define BIT_BROADCASTER_FIX_LEN	(3) // broadcaster_id                   8  uimsbf
									// reserved_future_use              4  bslbf
									// broadcaster_descriptors_length  12  uimsbf


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
	{
		adaptationHeaders.clear();
	}
	virtual ~CDsmccMessageHeader (void) {}


	uint8_t protocolDiscriminator;
	uint8_t dsmccType;
	uint16_t messageId;
	uint32_t transaction_id;
	uint8_t reserved;
	uint8_t adaptationLength;
	uint16_t messageLength;
	std::vector <CDsmccAdaptationHeader> adaptationHeaders;
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
	virtual ~CDownloadInfoIndication (void);

	
	CDsmccMessageHeader messageHeader;
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
	{
		adaptationHeaders.clear();
	}
	virtual ~CDsmccDownloadDataHeader (void) {}

	uint8_t protocolDiscriminator;
	uint8_t dsmccType;
	uint16_t messageId;
	uint32_t downloadId;
	uint8_t reserved;
	uint8_t adaptationLength;
	uint16_t messageLength;
	std::vector <CDsmccAdaptationHeader> adaptationHeaders;
};

// DDB
class CDownloadDataBlock
{
	CDownloadDataBlock (void)
		:moduleId (0)
		,moduleVersion (0)
		,reserved (0)
		,blockNumber (0)
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
};

class CDsmcc : public CSectionParser
{
public:
	class CTable {
	public:
		CTable (void) {};
		virtual ~CTable (void) {};


		ST_SECTION_HEADER header;

	};

public:
	explicit CDsmcc (size_t poolSize);
	virtual ~CDsmcc (void);

//	bool onSectionStarted (const CSectionInfo *pSection) override;
	void onSectionCompleted (const CSectionInfo *pCompSection) override;

	void dumpTables (void) const;
	void dumpTable (const CTable* pTable) const;
	void clear (void);

private:
	bool parse (const CSectionInfo *pCompSection, CTable* pOutTable);
	void releaseTables (void);

	std::vector <CTable*> mTables;
};

#endif
