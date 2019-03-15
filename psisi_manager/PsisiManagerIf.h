#ifndef _PSISI_MANAGER_IF_H_
#define _PSISI_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"


using namespace ThreadManager;

enum {
	EN_SEQ_PSISI_MANAGER_MODULE_UP = 0,
	EN_SEQ_PSISI_MANAGER_MODULE_DOWN,
	EN_SEQ_PSISI_MANAGER_CHECK_LOOP,
	EN_SEQ_PSISI_MANAGER_CHECK_PAT,
	EN_SEQ_PSISI_MANAGER_DUMP_TABLES,

	EN_SEQ_PSISI_MANAGER_NUM,
};

typedef enum {
	EN_PSISI_TYPE_PAT = 0,
	EN_PSISI_TYPE_PMT,
	EN_PSISI_TYPE_EIT_H,
	EN_PSISI_TYPE_NIT,
	EN_PSISI_TYPE_SDT,
	EN_PSISI_TYPE_RST,
	EN_PSISI_TYPE_BIT,

	EN_PSISI_TYPE_NUM,
} EN_PSISI_TYPE;


class CPsisiManagerIf : public CThreadMgrExternalIf
{
public:
	explicit CPsisiManagerIf (CThreadMgrExternalIf *pIf) : CThreadMgrExternalIf (pIf) {
	};

	virtual ~CPsisiManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_MODULE_DOWN);
	};

	bool reqCheckPAT (bool isCompleteNew) {
		bool _isCompNew = isCompleteNew;
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_CHECK_PAT, (uint8_t*)&_isCompNew, sizeof(_isCompNew));
	};

	bool reqDumpTables (EN_PSISI_TYPE type) {
		EN_PSISI_TYPE _type = type;
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_DUMP_TABLES, (uint8_t*)&_type, sizeof(_type));
	};


};

#endif
