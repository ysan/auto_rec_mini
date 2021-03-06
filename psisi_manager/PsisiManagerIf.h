#ifndef _PSISI_MANAGER_IF_H_
#define _PSISI_MANAGER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "Group.h"

#include "PsisiManagerStructs.h"


using namespace ThreadManager;

enum {
	EN_SEQ_PSISI_MANAGER__MODULE_UP = 0,
	EN_SEQ_PSISI_MANAGER__MODULE_DOWN,
	EN_SEQ_PSISI_MANAGER__GET_STATE,
	EN_SEQ_PSISI_MANAGER__CHECK_LOOP,					// inner
	EN_SEQ_PSISI_MANAGER__PARSER_NOTICE,				// inner
	EN_SEQ_PSISI_MANAGER__STABILIZATION_AFTER_TUNING,	// inner
	EN_SEQ_PSISI_MANAGER__REG_PAT_DETECT_NOTIFY,
	EN_SEQ_PSISI_MANAGER__UNREG_PAT_DETECT_NOTIFY,
	EN_SEQ_PSISI_MANAGER__REG_EVENT_CHANGE_NOTIFY,
	EN_SEQ_PSISI_MANAGER__UNREG_EVENT_CHANGE_NOTIFY,
	EN_SEQ_PSISI_MANAGER__REG_PSISI_STATE_NOTIFY,
	EN_SEQ_PSISI_MANAGER__UNREG_PSISI_STATE_NOTIFY,
	EN_SEQ_PSISI_MANAGER__GET_PAT_DETECT_STATE,
	EN_SEQ_PSISI_MANAGER__GET_CURRENT_SERVICE_INFOS,
	EN_SEQ_PSISI_MANAGER__GET_PRESENT_EVENT_INFO,
	EN_SEQ_PSISI_MANAGER__GET_FOLLOW_EVENT_INFO,
	EN_SEQ_PSISI_MANAGER__GET_CURRENT_NETWORK_INFO,
	EN_SEQ_PSISI_MANAGER__ENABLE_PARSE_EIT_SCHED,
	EN_SEQ_PSISI_MANAGER__DISABLE_PARSE_EIT_SCHED,
	EN_SEQ_PSISI_MANAGER__DUMP_CACHES,
	EN_SEQ_PSISI_MANAGER__DUMP_TABLES,

	EN_SEQ_PSISI_MANAGER__NUM,
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
	EN_PSISI_TYPE__CAT,
	EN_PSISI_TYPE__CDT,

	// for dump command
	EN_PSISI_TYPE__EIT_H_PF,
	EN_PSISI_TYPE__EIT_H_PF_simple,
	EN_PSISI_TYPE__EIT_H_SCHED,
	EN_PSISI_TYPE__EIT_H_SCHED_event,
	EN_PSISI_TYPE__EIT_H_SCHED_simple,

	EN_PSISI_TYPE_NUM,
} EN_PSISI_TYPE;

typedef enum {
	EN_PAT_DETECT_STATE__DETECTED = 0,
	EN_PAT_DETECT_STATE__NOT_DETECTED,

} EN_PAT_DETECT_STATE;

typedef enum {
	EN_PSISI_STATE__READY = 0,
	EN_PSISI_STATE__NOT_READY,

} EN_PSISI_STATE;


class CPsisiManagerIf : public CThreadMgrExternalIf, public CGroup
{
public:
	explicit CPsisiManagerIf (CThreadMgrExternalIf *pIf, uint8_t groupId=0)
		:CThreadMgrExternalIf (pIf)
		,CGroup (groupId)
	{
	};

	virtual ~CPsisiManagerIf (void) {
	};


	bool reqModuleUp (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER + getGroupId(), EN_SEQ_PSISI_MANAGER__MODULE_UP);
	};

	bool reqModuleDown (void) {
		return requestAsync (EN_MODULE_PSISI_MANAGER + getGroupId(), EN_SEQ_PSISI_MANAGER__MODULE_DOWN);
	};

	bool reqGetState (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_STATE
				);
	};

	bool reqGetStateSync (void) {
		return requestSync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_STATE
				);
	};

	bool reqRegisterPatDetectNotify (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__REG_PAT_DETECT_NOTIFY
				);
	};

	bool reqUnregisterPatDetectNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__UNREG_PAT_DETECT_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqRegisterEventChangeNotify (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__REG_EVENT_CHANGE_NOTIFY
				);
	};

	bool reqUnregisterEventChangeNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__UNREG_EVENT_CHANGE_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqRegisterPsisiStateNotify (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__REG_PSISI_STATE_NOTIFY
				);
	};

	bool reqUnregisterPsisiStateNotify (int client_id) {
		int _id = client_id;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__UNREG_PSISI_STATE_NOTIFY,
					(uint8_t*)&_id,
					sizeof(_id)
				);
	};

	bool reqGetPatDetectState (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_PAT_DETECT_STATE
				);
	};

	bool reqGetCurrentServiceInfos (PSISI_SERVICE_INFO *p_out_serviceInfos, int array_max_num) {
		if (!p_out_serviceInfos || array_max_num == 0) {
			return false;
		}

		REQ_SERVICE_INFOS_PARAM param = {p_out_serviceInfos, array_max_num};
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_CURRENT_SERVICE_INFOS,
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
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_PRESENT_EVENT_INFO,
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
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_FOLLOW_EVENT_INFO,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool reqGetCurrentNetworkInfo (PSISI_NETWORK_INFO *p_out_networkInfo) {
		if (!p_out_networkInfo) {
			return false;
		}

		REQ_NETWORK_INFO_PARAM param = {p_out_networkInfo};
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__GET_CURRENT_NETWORK_INFO,
					(uint8_t*)&param,
					sizeof(param)
				);
	};

	bool reqEnableParseEITSched (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__ENABLE_PARSE_EIT_SCHED
				);
	};

	bool reqDisableParseEITSched (void) {
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__DISABLE_PARSE_EIT_SCHED
				);
	};

	bool reqDumpCaches (int type) {
		int _type = type;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__DUMP_CACHES,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

	bool reqDumpTables (EN_PSISI_TYPE type) {
		EN_PSISI_TYPE _type = type;
		return requestAsync (
					EN_MODULE_PSISI_MANAGER + getGroupId(),
					EN_SEQ_PSISI_MANAGER__DUMP_TABLES,
					(uint8_t*)&_type,
					sizeof(_type)
				);
	};

};

#endif
