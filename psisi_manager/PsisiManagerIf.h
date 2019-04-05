#ifndef _PSISI_MANAGER_IF_H_
#define _PSISI_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrExternalIf.h"
#include "modules.h"

#include "PsisiManagerStructs.h"


using namespace ThreadManager;

enum {
	EN_SEQ_PSISI_MANAGER_MODULE_UP = 0,
	EN_SEQ_PSISI_MANAGER_MODULE_DOWN,
	EN_SEQ_PSISI_MANAGER_CHECK_LOOP,					// inner
	EN_SEQ_PSISI_MANAGER_PARSER_NOTICE,					// inner
	EN_SEQ_PSISI_MANAGER_STABILIZATION_AFTER_TUNING,	// inner
	EN_SEQ_PSISI_MANAGER_REG_PAT_DETECT_NOTIFY,
	EN_SEQ_PSISI_MANAGER_UNREG_PAT_DETECT_NOTIFY,
	EN_SEQ_PSISI_MANAGER_REG_EVENT_CHANGE_NOTIFY,
	EN_SEQ_PSISI_MANAGER_UNREG_EVENT_CHANGE_NOTIFY,
	EN_SEQ_PSISI_MANAGER_GET_CURRENT_SERVICE_INFOS,
	EN_SEQ_PSISI_MANAGER_GET_PRESENT_EVENT_INFO,
	EN_SEQ_PSISI_MANAGER_GET_FOLLOW_EVENT_INFO,
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
	EN_PSISI_NOTIFY__PAT_DETECTED = 0,
	EN_PSISI_NOTIFY__PAT_NOT_DETECTED,
	EN_PSISI_NOTIFY__EVENT_CHANGE,

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

	bool reqRegisterPatDetectNotify (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_REG_PAT_DETECT_NOTIFY
				);
	};

	bool reqUnregisterPatDetectNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_UNREG_PAT_DETECT_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqRegisterEventChangeNotify (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_REG_EVENT_CHANGE_NOTIFY
				);
	};

	bool reqUnregisterEventChangeNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_UNREG_EVENT_CHANGE_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqGetCurrentServiceInfos (PSISI_SERVICE_INFO *p_out_serviceInfos, int num) {
		if (!p_out_serviceInfos || num == 0) {
			return false;
		}

		REQ_SERVICE_INFO_PARAM param = {p_out_serviceInfos, num};
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_GET_CURRENT_SERVICE_INFOS,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool reqGetPresentEventInfo (PSISI_SERVICE_INFO *p_key, PSISI_EVENT_INFO *p_out_eventInfo) {
		if (!p_key || !p_out_eventInfo) {
			return false;
		}

		REQ_EVENT_INFO_PARAM param = {*p_key, p_out_eventInfo};
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_GET_PRESENT_EVENT_INFO,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool reqGetFollowEventInfo (PSISI_SERVICE_INFO *p_key, PSISI_EVENT_INFO *p_out_eventInfo) {
		if (!p_key || !p_out_eventInfo) {
			return false;
		}

		REQ_EVENT_INFO_PARAM param = {*p_key, p_out_eventInfo};
		return requestAsync (
					EN_MODULE_PSISI_MANAGER,
					EN_SEQ_PSISI_MANAGER_GET_FOLLOW_EVENT_INFO,
					(uint8_t*)&param,
					sizeof(param)
				);
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
