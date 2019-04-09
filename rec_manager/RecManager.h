#ifndef _REC_MANAGER_H_
#define _REC_MANAGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Utils.h"
#include "RecManagerIf.h"

#include "TunerControlIf.h"
#include "PsisiManagerIf.h"


using namespace ThreadManager;


typedef enum {
	EN_REC_STATE__INIT = 0,
	EN_REC_STATE__PRE_PROC,
	EN_REC_STATE__NOW_RECORDING,
	EN_REC_STATE__POST_PROC,
} EN_REC_STATE;


typedef struct {

	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;

	uint16_t event_id;
	CEtime start_time;
	CEtime end_time;

	char title_name [1024];

	void dump (void) {
		_UTL_LOG_I (
			"tsid:[0x%04x] org_nid:[0x%04x] svcid:[0x%04x] evtid:[0x%04x]",
			transport_stream_id,
			original_network_id,
			service_id,
			event_id
		);
		_UTL_LOG_I (
			"time:[%s - %s]",
			start_time.toString(),
			end_time.toString()
		);
		_UTL_LOG_I ("[%s]", title_name);
	}

} _RESERVE;


class CRecManager
	:public CThreadMgrBase
	,public CTunerControlIf::ITsReceiveHandler
{
public:
	CRecManager (char *pszName, uint8_t nQueNum);
	virtual ~CRecManager (void);


	void moduleUp (CThreadMgrIf *pIf);
	void moduleDown (CThreadMgrIf *pIf);
	void reserveCheck (CThreadMgrIf *pIf);
	void startRecording (CThreadMgrIf *pIf);


private:
	void onReceiveNotify (CThreadMgrIf *pIf) override;


	// CTunerControlIf::ITsReceiveHandler
	bool onPreTsReceive (void) override;
	void onPostTsReceive (void) override;
	bool onCheckTsReceiveLoop (void) override;
	bool onTsReceived (void *p_ts_data, int length) override;



	ST_SEQ_BASE mSeqs [EN_SEQ_REC_MANAGER_NUM]; // entity

	uint8_t m_tunerNotify_clientId;
	int m_tsReceive_handlerId;
	uint8_t m_eventChangeNotify_clientId;

	EN_REC_STATE m_recState;
};

#endif
