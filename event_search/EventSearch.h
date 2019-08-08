#ifndef _EVENT_SEARCH_H_
#define _EVENT_SEARCH_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "threadmgr_if.h"
#include "threadmgr_util.h"

#include "ThreadMgrIf.h"
#include "ThreadMgrExternalIf.h"

#include "ThreadMgrBase.h"
#include "ThreadMgr.h"

#include "Utils.h"
#include "TsAribCommon.h"
#include "EventSearchIf.h"

#include "RecManagerIf.h"
#include "EventScheduleManagerIf.h"

#include "cereal/cereal.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/archives/json.hpp"


using namespace ThreadManager;


class CEventSearch : public CThreadMgrBase
{
public:
	CEventSearch (char *pszName, uint8_t nQueNum);
	virtual ~CEventSearch (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_addRecReserve_keywordSearch (CThreadMgrIf *pIf);

	void onReceiveNotify (CThreadMgrIf *pIf) override;


private:

	void dumpEventNameKeywords (void) const;
	void dumpExtendedEventKeywords (void) const;

	void saveEventNameKeywords (void);
	void loadEventNameKeywords (void);

	void saveExtendedEventKeywords (void);
	void loadExtendedEventKeywords (void);


	ST_SEQ_BASE mSeqs [EN_SEQ_EVENT_SEARCH__NUM]; // entity

	uint8_t m_cache_sched_state_notify_client_id;
	std::vector <std::string> m_event_name_keywords;
	std::vector <std::string> m_extended_event_keywords;

};

#endif
