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
#include "PsisiManagerIf.h"

#include "Utils.h"
#include "modules.h"


using namespace ThreadManager;


int main (void)
{
	initLogStdout();


	CThreadMgr *p_threadmgr = CThreadMgr::getInstance();

	if (!p_threadmgr->setup (getModules(), EN_MODULE_NUM)) {
		exit (EXIT_FAILURE);
	}

	p_threadmgr->getExternalIf()->createExternalCp();


	CCommandServerIf *p_comSvrIf = new CCommandServerIf (p_threadmgr->getExternalIf());
	CTunerControlIf *p_tunerCtlIf = new CTunerControlIf (p_threadmgr->getExternalIf());
	CPsisiManagerIf *p_psisiMgrIf = new CPsisiManagerIf (p_threadmgr->getExternalIf());


	p_comSvrIf-> reqModuleUp ();
	p_tunerCtlIf-> reqModuleUp ();
	p_psisiMgrIf->reqModuleUp();



	pause ();


	p_threadmgr->teardown();
	delete p_threadmgr;
	p_threadmgr = NULL;


	exit (EXIT_SUCCESS);
}
