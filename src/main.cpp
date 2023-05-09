#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <utility>
#include <memory>

#include "http_server.h"

#include "ThreadMgrpp.h"

#include "CommandServerIf.h"
#include "TunerControlIf.h"
#include "PsisiManagerIf.h"
#include "TunerServiceIf.h"
#include "RecManagerIf.h"
#include "ChannelManagerIf.h"
#include "EventScheduleManagerIf.h"
#include "EventSearchIf.h"
#include "ViewingManagerIf.h"

#include "threadmgr_util.h"
#include "tssplitter_lite.h"

#include "Utils.h"
#include "Forker.h"
#include "modules.h"
#include "Settings.h"

#include "StreamHandler.h"


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
			printf ("mkdir [%s]\n", change_directory_path.c_str());
			int r = CUtils::makedir (change_directory_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
			if (r == 0) {
				printf ("mkdir success [%s]\n", change_directory_path.c_str());
			} else {
				printf ("mkdir failure [%s]\n", change_directory_path.c_str());
				exit (EXIT_FAILURE);
			}
		}
		printf ("chdir [%s]\n", change_directory_path.c_str());
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
	CSettings *s = CSettings::get_instance ();
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

	if (s->get_params().is_syslog_output()) {
		auto syslog = std::make_shared<CSyslog> ("/dev/log", LOG_USER, "auto_rec_mini");
		s_logger.set_syslog(syslog);
	}

	split_set_printf_cb (splitter_log);


	CUtils::set_shared(&s_shared);
	s->get_params().dump ();


	// ----- setup thread manager -----
	threadmgr::CThreadMgr *mgr = threadmgr::CThreadMgr::get_instance();
	if (!mgr->setup (modules::get_modules(), static_cast<int>(modules::module_id::max))) {
		exit (EXIT_FAILURE);
	}

	mgr->get_external_if()->create_external_cp();


	CCommandServerIf com_svr_if (mgr->get_external_if());
	std::unique_ptr<CTunerControlIf> tuner_ctl_if[CGroup::GROUP_MAX];
	std::unique_ptr<CPsisiManagerIf> psisi_mgr_if [CGroup::GROUP_MAX];
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		tuner_ctl_if [_gr] = std::unique_ptr<CTunerControlIf>(new CTunerControlIf(mgr->get_external_if(), _gr));
		psisi_mgr_if [_gr] = std::unique_ptr<CPsisiManagerIf>(new CPsisiManagerIf(mgr->get_external_if(), _gr));
	}
	CTunerServiceIf tuner_svc_if (mgr->get_external_if());
	CRecManagerIf rec_mgr_if (mgr->get_external_if());
	CChannelManagerIf ch_mgr_if (mgr->get_external_if());
	CEventScheduleManagerIf sched_mgr_if (mgr->get_external_if());
	CEventSearchIf search_if (mgr->get_external_if());
	CViewingManagerIf view_mgr_if (mgr->get_external_if());


//	uint32_t opt = mgr->get_external_if()->get_request_option ();
//	opt |= REQUEST_OPTION__WITH_TIMEOUT_MSEC;
//	opt &= 0x0000ffff; // clear timeout val
//	opt |= 1000 << 16; // set timeout 1sec
	// set without-reply
//	opt |= REQUEST_OPTION__WITHOUT_REPLY;
//	mgr->get_external_if()->set_request_option (opt);

	// ----- modules up -----
	{
		com_svr_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("commnd server module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("commnd server module up done.");
	}
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		tuner_ctl_if[_gr]->request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("tuner control %d module up failed.", _gr);
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("tuner control %d module up done.", _gr);
	}
	for (uint8_t _gr = 0; _gr < CGroup::GROUP_MAX; ++ _gr) {
		psisi_mgr_if[_gr]->request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("psisi manager %d module up failed.", _gr);
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("psisi manager %d module up done.", _gr);
	}
	{
		tuner_svc_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("tuner service module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("tuner service module up done.");
	}
	{
		rec_mgr_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("rec manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("rec manager module up done.");
	}
	{
		ch_mgr_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("channel manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("channel manager module up done.");
	}
	{
		sched_mgr_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("event schedule manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("event schedule manager module up done.");
	}
	{
		search_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("event search module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("event search module up done.");
	}
	{
		view_mgr_if.request_module_up();
		threadmgr::CSource& r = mgr->get_external_if()-> receive_external();
		if (r.get_result() != threadmgr::result::success) {
			_UTL_LOG_E ("viewing manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("viewing manager module up done.");
	}
	
	// reset without-reply
//	opt &= ~REQUEST_OPTION__WITH_TIMEOUT_MSEC;
//	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
//	mgr->get_external_if()->set_request_option (opt);

	if (!stream_handler_funcs::setup_instance(mgr->get_external_if())) {
		_UTL_LOG_E ("stream handlers setup_instance failed.");
	}
	_UTL_LOG_I ("stream handlers setup_instance done.");

	uint16_t port = CSettings::get_instance()->get_params().get_http_server_port();
	http_server http{port, mgr->get_external_if()};
	std::thread t([&http]{
		http.up();
	});


	mgr->wait ();


	http.down();
	t.join();

	mgr->get_external_if()->destroy_external_cp();
	mgr->teardown();


	exit (EXIT_SUCCESS);
}
