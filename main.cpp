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

#include "modules.h"


using namespace ThreadManager;


int main (void)
{
	initLogStdout();


	CThreadMgr *p_thread_mgr = CThreadMgr::getInstance();

	if (!p_thread_mgr->setup (getModules(), EN_MODULE_NUM)) {
		exit (EXIT_FAILURE);
	}

	CCommandServerIf *p_command_server_if = new CCommandServerIf (p_thread_mgr->getExternalIf());


	p_thread_mgr->getExternalIf()->createExternalCp();


	p_command_server_if-> reqStart ();



	pause ();




	p_thread_mgr->teardown();
	delete p_thread_mgr;
	p_thread_mgr = NULL;


	exit (EXIT_SUCCESS);
}
