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

#include "CommandServerIf.h"
#include "TunerControlIf.h"

#include "Utils.h"
#include "modules.h"


using namespace ThreadManager;


class CTsTest : public CTunerControlIf::ITsReceiveHandler {
public:
	CTsTest (void) {}
	virtual ~CTsTest (void) {}

	bool onPreTsReceive (void) {
		_UTL_LOG_I (__PRETTY_FUNCTION__);
		return true;
	}

	void onPostTsReceive (void) {
		_UTL_LOG_I (__PRETTY_FUNCTION__);
	}

	bool onCheckTsReceiveLoop (void) {
		_UTL_LOG_I (__PRETTY_FUNCTION__);
		return true;
	}

	bool onTsReceived (void *p_ts_data, int length) {
		_UTL_LOG_I (__PRETTY_FUNCTION__);
		return true;
	}
};


int main (void)
{
	initLogStdout();


	CThreadMgr *p_threadmgr = CThreadMgr::getInstance();

	if (!p_threadmgr->setup (getModules(), EN_MODULE_NUM)) {
		exit (EXIT_FAILURE);
	}

	CCommandServerIf *p_comSvrIf = new CCommandServerIf (p_threadmgr->getExternalIf());
	CTunerControlIf *p_tunerCtlIf = new CTunerControlIf (p_threadmgr->getExternalIf());


	p_threadmgr->getExternalIf()->createExternalCp();


	p_comSvrIf-> reqModuleUp ();
	p_tunerCtlIf-> reqModuleUp ();


	CTsTest *p_test = new CTsTest();
	_UTL_LOG_I ("p_test %p\n", p_test);
	p_tunerCtlIf-> reqRegisterTsReceiveHandler ((CTunerControlIf::ITsReceiveHandler**)&p_test);
	ST_THM_SRC_INFO* r = p_threadmgr->getExternalIf()-> receiveExternal();
	_UTL_LOG_I (" rslt:[%d] id:[%d]", r->enRslt, *(int*)r->msg.pMsg);



	pause ();


	p_threadmgr->teardown();
	delete p_threadmgr;
	p_threadmgr = NULL;


	exit (EXIT_SUCCESS);
}
