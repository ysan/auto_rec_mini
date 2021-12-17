#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include "ThreadMgrpp.h"

#include "CommandServerIf.h"
#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "TunerServiceIf.h"
#include "RecManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"
#include "EventSearchIf.h"

#include "Utils.h"
#include "modules.h"
#include "Settings.h"


using namespace ThreadManager;

static void _usage (char* _arg_0)
{
	printf ("\n");
	printf ("Usage: %s -c conf-file [OPTIONS]\n", _arg_0);
	printf ("  OPTIONS\n");
	printf ("      -c conf-file, --conf=conf-file  (*requiredment option*)\n");
	printf ("        Specify a configuration file. (settings.json)\n\n");
	printf ("      -d change-directory, --dir=change-directory\n");
	printf ("        Specify the process starting directory.\n\n");
	printf ("      -h\n");
	printf ("        print usage.\n\n");
}

int main (int argc, char *argv[])
{
	int c = 0;
	std::string settings_json_path = "";
	std::string change_directory_path = "";

	while (1) {
		int option_index = 0;
		struct option long_options [] = {
			{"conf" , required_argument, 0, 'c'}, // load conf file
			{"dir",   required_argument, 0, 'd'}, // change directory
			{0, 0, 0, 0},
		};

		c = getopt_long(argc, argv, "c:d:h", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 'c':
			settings_json_path = optarg;
			break;

		case 'd':
			change_directory_path = optarg;
			break;

		case 'h':
			_usage (argv[0]);
			exit (EXIT_SUCCESS);
			break;

		default:
			_usage (argv[0]);
			exit (EXIT_FAILURE);
		}
	}

	if (settings_json_path.length() == 0) {
		printf ("must specify configuration file. (settings.json)\nrerun set '-c' option.\n");
		_usage (argv[0]);
		exit (EXIT_FAILURE);
	}

	if (change_directory_path.length() > 0) {
		printf ("chdir %s\n", change_directory_path.c_str());
		chdir (change_directory_path.c_str());
	}


//	if (argc != 2) {
//		printf ("unexpected arguments.\n");
//		exit (EXIT_FAILURE);
//	}

//	std::string settings_json = argv[1];
	CSettings *s = CSettings::getInstance ();
	if (!s->load (settings_json_path)) {
		printf ("invalid settings.json.\n");
		exit (EXIT_FAILURE);
	}


	initLogStdout(); // threadmgr log init

	// syslog initialize
	if (s->getParams()->isSyslogOutput()) {
		initSyslog(); // threadmgr syslog output
		CUtils::initSyslog();
	}


	s->getParams()->dump ();


	CThreadMgr *p_mgr = CThreadMgr::getInstance();
	if (!p_mgr->setup (getModules(), EN_MODULE_NUM)) {
		exit (EXIT_FAILURE);
	}

	p_mgr->getExternalIf()->createExternalCp();


	CCommandServerIf *p_comSvrIf = new CCommandServerIf (p_mgr->getExternalIf());
#if 0
	CTunerControlIf *p_tunerCtlIf = new CTunerControlIf (p_mgr->getExternalIf());
	CPsisiManagerIf *p_psisiMgrIf = new CPsisiManagerIf (p_mgr->getExternalIf());
#else
	CTunerControlIf *p_tunerCtlIf [CGroup::GROUP_MAX];
	CPsisiManagerIf *p_psisiMgrIf [CGroup::GROUP_MAX];
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		p_tunerCtlIf [_gr] = new CTunerControlIf(p_mgr->getExternalIf(), _gr);
		p_psisiMgrIf [_gr] = new CPsisiManagerIf(p_mgr->getExternalIf(), _gr);
	}
#endif
	CTunerServiceIf *p_tunerSvcIf = new CTunerServiceIf (p_mgr->getExternalIf());
	CRecManagerIf *p_recMgrIf = new CRecManagerIf (p_mgr->getExternalIf());
	CChannelManagerIf *p_chMgrIf = new CChannelManagerIf (p_mgr->getExternalIf());
	CEventScheduleManagerIf *p_schedMgrIf = new CEventScheduleManagerIf (p_mgr->getExternalIf());
	CEventSearchIf *p_searchIf = new CEventSearchIf (p_mgr->getExternalIf());


	uint32_t opt = p_mgr->getExternalIf()->getRequestOption ();
//	opt |= REQUEST_OPTION__WITH_TIMEOUT_MSEC;
//	opt &= 0x0000ffff; // clear timeout val
//	opt |= 1000 << 16; // set timeout 1sec
	opt |= REQUEST_OPTION__WITHOUT_REPLY;
	p_mgr->getExternalIf()->setRequestOption (opt);


	// modules up
	p_comSvrIf-> reqModuleUp ();
#if 0
	p_tunerCtlIf-> reqModuleUp ();
	p_psisiMgrIf->reqModuleUp();
#else
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		p_tunerCtlIf[_gr]->reqModuleUp();
		p_psisiMgrIf[_gr]->reqModuleUp();
	}
#endif
	p_tunerSvcIf->reqModuleUp();
	p_recMgrIf->reqModuleUp();
	p_chMgrIf->reqModuleUp();
	p_schedMgrIf->reqModuleUp();
	p_searchIf->reqModuleUp();



	p_mgr->wait ();


	p_mgr->getExternalIf()->destroyExternalCp();
	p_mgr->teardown();


	// syslog finalize
	if (s->getParams()->isSyslogOutput()) {
		finalizSyslog(); // threadmgr syslog output
		CUtils::finalizSyslog();
	}


	exit (EXIT_SUCCESS);
}
