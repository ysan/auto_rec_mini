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
	EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,
	EN_SEQ_PSISI_MANAGER_STABILIZATION_AFTER_TUNING,
	EN_SEQ_PSISI_MANAGER_REG_PSISI_NOTIFY,
	EN_SEQ_PSISI_MANAGER_UNREG_PSISI_NOTIFY,
	EN_SEQ_PSISI_MANAGER_DUMP_CACHES,
	EN_SEQ_PSISI_MANAGER_DUMP_TABLES,

	EN_SEQ_PSISI_MANAGER_NUM,
};

typedef enum {
	EN_PSISI_TYPE__PAT = 0,
	EN_PSISI_TYPE__PMT,
	EN_PSISI_TYPE__EIT_H,
	EN_PSISI_TYPE__EIT_M,
	EN_PSISI_TYPE__EIT_L,
	EN_PSISI_TYPE__NIT,
	EN_PSISI_TYPE__SDT,
	EN_PSISI_TYPE__RST,
	EN_PSISI_TYPE__BIT,

	// for dump command
	EN_PSISI_TYPE__EIT_H_PF,
	EN_PSISI_TYPE__EIT_H_PF_simple,
	EN_PSISI_TYPE__EIT_H_SCH,
	EN_PSISI_TYPE__EIT_H_SCH_simple,

	EN_PSISI_TYPE_NUM,
} EN_PSISI_TYPE;

typedef enum {
	EN_PSISI_NOTIFY__EVENT_CHANGE = 0,

} EN_PSISI_NOTIFY;



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

	bool reqRegisterPsisiNotify (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_REG_PSISI_NOTIFY);
	};

	bool reqUnregisterPsisiNotify (int client_id) {
		int _id = client_id;
		return requestAsync (EN_MODULE_PSISI_MANAGER, EN_SEQ_PSISI_MANAGER_UNREG_PSISI_NOTIFY, (uint8_t*)&_id, sizeof(_id));
	};

	bool reqDumpCaches (int type) {
		int _type = type;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_DUMP_CACHES,
					(uint8_t*)&_type,
					sizeof(_type)
			);
	};

	bool reqDumpTables (EN_PSISI_TYPE type) {
		EN_PSISI_TYPE _type = type;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_DUMP_TABLES,
					(uint8_t*)&_type,
					sizeof(_type)
			);
	};


};

#endif
