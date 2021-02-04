#ifndef _TUNER_SERVICE_H_
#define _TUNER_SERVICE_H_

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ThreadMgrpp.h"

#include "Utils.h"
#include "Group.h"
#include "Settings.h"
#include "TunerServiceIf.h"

using namespace ThreadManager;


class CTunerService : public CThreadMgrBase
{
public:
	typedef struct _resource {
		_resource (void) 
			:tuner_id (0xff)
			,client (CTunerServiceIf::client::OTHER)
		{
		}

		virtual ~_resource (void) {
		}

		uint8_t tuner_id;
		CTunerServiceIf::client client;
	} RESOURCE_t;

public:
	CTunerService (char *pszName, uint8_t nQueNum);
	virtual ~CTunerService (void);


	void onReq_moduleUp (CThreadMgrIf *pIf);
	void onReq_moduleDown (CThreadMgrIf *pIf);
	void onReq_open (CThreadMgrIf *pIf);
	void onReq_close (CThreadMgrIf *pIf);

private:

	int m_tuner_resource_max;
	std::vector <std::shared_ptr<RESOURCE_t>> m_resource_allcates;

};

#endif
