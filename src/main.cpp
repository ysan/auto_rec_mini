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
#include <chrono>
#include <signal.h>

#include "http_server.h"

#include "coseqpp.h"

#include "command_server_if.h"
#include "tuner_control_if.h"
#include "psisi_manager_if.h"
#include "tuner_service_if.h"
#include "rec_manager_if.h"
#include "channel_manager_if.h"
#include "event_schedule_manager_if.h"
#include "event_search_if.h"
#include "viewing_manager_if.h"

#include "tssplitter_lite.h"

#include "utils.h"
#include "forker.h"
#include "modules.h"
#include "settings.h"

#include "stream_handler.h"


static arec::common::logger s_logger;
static arec::common::shared<> s_shared;

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

// coseq 内部ログを app ロガーへ流すコールバック (旧 threadmgr_log / _LW 相当)
static void coseq_log_to_app (coseq::log_level level, const std::string &msg)
{
	switch (level) {
	case coseq::log_level::debug:
		_UTL_LOG_D ("%s", msg.c_str());
		break;
	case coseq::log_level::info:
		_UTL_LOG_I ("%s", msg.c_str());
		break;
	case coseq::log_level::warn:
		_UTL_LOG_W ("%s", msg.c_str());
		break;
	case coseq::log_level::error:
		_UTL_LOG_E ("%s", msg.c_str());
		break;
	default:
		break;
	}
}

// SIGINT/SIGTERM でシャットダウン要求(v1 の wait_all 解除 相当のトリガ)
static void on_shutdown_signal (int)
{
	modules::request_shutdown();
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
			int r = arec::common::utils::makedir (change_directory_path.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
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
	arec::settings *s = arec::settings::get_instance ();
	if (!s->load (settings_json_path)) {
		printf ("Invalid settings.json.\n");
		exit (EXIT_FAILURE);
	}


	// ----- log settings -----
	s_logger.set_log_level(arec::common::logger::level::info);
	s_logger.append_handler(stdout);
	arec::common::utils::set_logger(&s_logger);

	coseq::set_log_cb (coseq_log_to_app);

	if (s->get_params().is_syslog_output()) {
		auto syslog = std::make_shared<arec::common::syslog_util> ("/dev/log", LOG_USER, "auto_rec_mini");
		s_logger.set_syslog(syslog);
	}

	split_set_printf_cb (splitter_log);


	arec::common::utils::set_shared(&s_shared);
	s->get_params().dump ();


	// ----- setup thread manager -----
	coseq::manager mgr;
	if (!mgr.setup (modules::get_modules())) {
		exit (EXIT_FAILURE);
	}
	modules::set_manager (&mgr);


	arec::command_server::client_iface com_svr_if (&mgr);
	std::unique_ptr<arec::tuner_control::client_iface> tuner_ctl_if[arec::group::GROUP_MAX];
	std::unique_ptr<arec::psisi::client_iface> psisi_mgr_if [arec::group::GROUP_MAX];
	for (uint8_t _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		tuner_ctl_if [_gr] = std::unique_ptr<arec::tuner_control::client_iface>(new arec::tuner_control::client_iface(&mgr, _gr));
		psisi_mgr_if [_gr] = std::unique_ptr<arec::psisi::client_iface>(new arec::psisi::client_iface(&mgr, _gr));
	}
	arec::tuner_service::client_iface tuner_svc_if (&mgr);
	arec::rec::client_iface rec_mgr_if (&mgr);
	arec::channel::client_iface ch_mgr_if (&mgr);
	arec::event_schedule::client_iface sched_mgr_if (&mgr);
	arec::event_search::client_iface search_if (&mgr);
	arec::viewing::client_iface view_mgr_if (&mgr);


//	uint32_t opt = mgr->get_external_if()->get_request_option ();
//	opt |= REQUEST_OPTION__WITH_TIMEOUT_MSEC;
//	opt &= 0x0000ffff; // clear timeout val
//	opt |= 1000 << 16; // set timeout 1sec
	// set without-reply
//	opt |= REQUEST_OPTION__WITHOUT_REPLY;
//	mgr->get_external_if()->set_request_option (opt);

	// ----- modules up -----
	// request_module_up() は request_sync 化済み(完了を待って bool を返す)
	{
		if (!com_svr_if.request_module_up()) {
			_UTL_LOG_E ("commnd server module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("commnd server module up done.");
	}
	for (uint8_t _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		if (!tuner_ctl_if[_gr]->request_module_up()) {
			_UTL_LOG_E ("tuner control %d module up failed.", _gr);
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("tuner control %d module up done.", _gr);
	}
	for (uint8_t _gr = 0; _gr < arec::group::GROUP_MAX; ++ _gr) {
		if (!psisi_mgr_if[_gr]->request_module_up()) {
			_UTL_LOG_E ("psisi manager %d module up failed.", _gr);
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("psisi manager %d module up done.", _gr);
	}
	{
		if (!tuner_svc_if.request_module_up()) {
			_UTL_LOG_E ("tuner service module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("tuner service module up done.");
	}
	{
		if (!rec_mgr_if.request_module_up()) {
			_UTL_LOG_E ("rec manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("rec manager module up done.");
	}
	{
		if (!ch_mgr_if.request_module_up()) {
			_UTL_LOG_E ("channel manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("channel manager module up done.");
	}
	{
		if (!sched_mgr_if.request_module_up()) {
			_UTL_LOG_E ("event schedule manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("event schedule manager module up done.");
	}
	{
		if (!search_if.request_module_up()) {
			_UTL_LOG_E ("event search module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("event search module up done.");
	}
	{
		if (!view_mgr_if.request_module_up()) {
			_UTL_LOG_E ("viewing manager module up failed.");
			exit (EXIT_FAILURE);
		}
		_UTL_LOG_I ("viewing manager module up done.");
	}

	// reset without-reply
//	opt &= ~REQUEST_OPTION__WITH_TIMEOUT_MSEC;
//	opt &= ~REQUEST_OPTION__WITHOUT_REPLY;
//	mgr->get_external_if()->set_request_option (opt);

	if (!stream_handler_funcs::setup_instance(&mgr)) {
		_UTL_LOG_E ("stream handlers setup_instance failed.");
	}
	_UTL_LOG_I ("stream handlers setup_instance done.");

	uint16_t port = arec::settings::get_instance()->get_params().get_http_server_port();
	http_server http{port, &mgr};
	std::thread t([&http]{
		http.up();
	});


	// SIGINT/SIGTERM をシャットダウン契機に
	signal (SIGINT,  on_shutdown_signal);
	signal (SIGTERM, on_shutdown_signal);

	// v1 の mgr->wait() 相当: シャットダウン要求(シグナル / command_server 等)を待つ
	while (!modules::is_shutdown_requested()) {
		std::this_thread::sleep_for (std::chrono::milliseconds (200));
	}


	http.down();
	t.join();

	mgr.teardown();


	exit (EXIT_SUCCESS);
}
