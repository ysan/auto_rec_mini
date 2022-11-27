#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <utility>

#include "ThreadMgrpp.h"

#include "CommandServerIf.h"
#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "TunerServiceIf.h"
#include "RecManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"
#include "EventSearchIf.h"

#include "threadmgr_util.h"
#include "tssplitter_lite.h"

#include "Utils.h"
#include "Forker.h"
#include "modules.h"
#include "Settings.h"


using namespace ThreadManager;

static CLogger s_logger;
static CShared<> s_shared;

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

void threadmgr_log (
	FILE *pFp,
	EN_LOG_TYPE enLogType,
	const char *pszFile,
	const char *pszFunc,
	int nLine,
	const char *pszFormat,
	...
)
{
	char buff [256] = {0};
	va_list va;
	va_start (va, pszFormat);
	vsnprintf (buff, sizeof(buff), pszFormat, va);
	switch ((int)enLogType) {
	case EN_LOG_TYPE_D:
		_UTL_LOG_D ("(%s,%s(),%d) %s", pszFile, pszFunc, nLine, buff);
		break;
	case EN_LOG_TYPE_I:
		_UTL_LOG_I ("(%s,%s(),%d) %s", pszFile, pszFunc, nLine, buff);
		break;
	case EN_LOG_TYPE_W:
		_UTL_LOG_W ("(%s,%s(),%d) %s", pszFile, pszFunc, nLine, buff);
		break;
	case EN_LOG_TYPE_E:
		_UTL_LOG_E ("(%s,%s(),%d) %s", pszFile, pszFunc, nLine, buff);
		break;
	case EN_LOG_TYPE_PE:
		_UTL_PERROR ("(%s,%s(),%d) %s", pszFile, pszFunc, nLine, buff);
		break;
	default:
		break;
	}
	va_end (va);
}

void threadmgr_log_lw (
	FILE *pFp,
	EN_LOG_TYPE enLogType,
	const char *pszFormat,
	...
)
{
	char buff [256] = {0};
	va_list va;
	va_start (va, pszFormat);
	vsnprintf (buff, sizeof(buff), pszFormat, va);
	switch ((int)enLogType) {
	case EN_LOG_TYPE_D:
		_UTL_LOG_D (buff);
		break;
	case EN_LOG_TYPE_I:
		_UTL_LOG_I (buff);
		break;
	case EN_LOG_TYPE_W:
		_UTL_LOG_W (buff);
		break;
	case EN_LOG_TYPE_E:
		_UTL_LOG_E (buff);
		break;
	case EN_LOG_TYPE_PE:
		_UTL_PERROR (buff);
		break;
	default:
		break;
	}
	va_end (va);
}

int splitter_log (FILE *fp, const char* format, ...)
{
	char buff [128] = {0};
	va_list va;
	va_start (va, format);
	vsnprintf (buff, sizeof(buff), format, va);
	if (fp == stderr) {
		_UTL_LOG_E (buff);
	} else {
		_UTL_LOG_I (buff);
	}
	va_end (va);
	return 0;
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
		printf ("Must specify configuration file. (settings.json)\nrerun set '-c' option.\n");
		_usage (argv[0]);
		exit (EXIT_FAILURE);
	}

	// ----- change directory -----
	if (change_directory_path.length() > 0) {
		struct stat _s;
		if (stat(change_directory_path.c_str(), &_s) != 0) {
			CForker forker;
			if (!forker.create_pipes()) {
				printf ("forker.create_pipes failure\n");
				exit (EXIT_FAILURE);
			}
			std::string s = "/bin/mkdir -p " + change_directory_path;
			printf ("[%s]", s.c_str());
			if (!forker.do_fork(std::move(s))) {
				printf ("forker.do_fork failure\n");
				exit (EXIT_FAILURE);
			}

			CForker::CChildStatus cs = forker.wait_child();
			printf ("is_normal_end %d  get_return_code %d", cs.is_normal_end(), cs.get_return_code());
			forker.destroy_pipes();
			if (cs.is_normal_end() && cs.get_return_code() == 0) {
				// success
				printf ("mkdir -p %s\n", change_directory_path.c_str());
			} else {
				printf ("mkdir failure [mkdir -p %s]\n", change_directory_path.c_str());
				exit (EXIT_FAILURE);
			}
		}
		printf ("chdir %s\n", change_directory_path.c_str());
		chdir (change_directory_path.c_str());
	}


	{
		struct stat _s;
		if (stat(settings_json_path.c_str(), &_s) != 0) {
			printf ("Not exists settings.json. [%s]\n", settings_json_path.c_str());
			exit (EXIT_FAILURE);
		}
	}


	// ----- load settings.json -----
	CSettings *s = CSettings::getInstance ();
	if (!s->load (settings_json_path)) {
		printf ("Invalid settings.json.\n");
		exit (EXIT_FAILURE);
	}


	// ----- log settings -----
	s_logger.set_log_level(CLogger::level::info);
	s_logger.append_handler(stdout);
	CUtils::set_logger(&s_logger);

	setAlternativeLog (threadmgr_log);
	setAlternativeLogLW (threadmgr_log_lw);

	if (s->getParams()->isSyslogOutput()) {
		auto syslog = std::make_shared<CSyslog> ("/dev/log", LOG_USER, "auto_rec_mini");
		s_logger.set_syslog(syslog);
	}

	split_set_printf_cb (splitter_log);


	CUtils::set_shared(&s_shared);
	s->getParams()->dump ();


	// ----- setup thread manager -----
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


	delete p_comSvrIf;
#if 0
	delete p_tunerCtlIf;
	delete p_psisiMgrIf;
#else
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		delete p_tunerCtlIf[_gr];
		delete p_psisiMgrIf[_gr];
	}
#endif
	delete p_tunerSvcIf;
	delete p_recMgrIf;
	delete p_chMgrIf;
	delete p_schedMgrIf;
	delete p_searchIf;


	exit (EXIT_SUCCESS);
}
