#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <atomic>

#include "command_server.h"
#include "tune_thread.h"
#include "tuner_control.h"
#include "psisi_manager.h"
#include "tuner_service.h"
#include "rec_manager.h"
#include "channel_manager.h"
#include "event_schedule_manager.h"
#include "event_search.h"
#include "viewing_manager.h"

#include "modules.h"
#include "viewing_manager/viewing_manager.h"


namespace modules {

// module_id の並び順で登録する(setup が vector の index を module idx に割り当てるため)
static std::vector<std::shared_ptr<coseq::module_base>> s_modules;

static void build_modules (void)
{
	if (!s_modules.empty()) {
		return;
	}

	s_modules = {
		std::make_shared<arec::tuner_control::tune_thread>          ((char*)"TuneTh_0"            , 10, 0),  // group0
		std::make_shared<arec::tuner_control::tune_thread>          ((char*)"TuneTh_1"            , 10, 1),  // group1
		std::make_shared<arec::tuner_control::tune_thread>          ((char*)"TuneTh_2"            , 10, 2),  // group2
		std::make_shared<arec::tuner_control::tune_thread>          ((char*)"TuneTh_3"            , 10, 3),  // group3

		std::make_shared<arec::tuner_control::manager>        ((char*)"TunerCtl_0"          , 10, 0),  // group0
		std::make_shared<arec::tuner_control::manager>        ((char*)"TunerCtl_1"          , 10, 1),  // group1
		std::make_shared<arec::tuner_control::manager>        ((char*)"TunerCtl_2"          , 10, 2),  // group2
		std::make_shared<arec::tuner_control::manager>        ((char*)"TunerCtl_3"          , 10, 3),  // group3

		std::make_shared<arec::psisi::manager>        ((char*)"PsisiMgr_0"          , 100, 0), // group0
		std::make_shared<arec::psisi::manager>        ((char*)"PsisiMgr_1"          , 100, 1), // group1
		std::make_shared<arec::psisi::manager>        ((char*)"PsisiMgr_2"          , 100, 2), // group2
		std::make_shared<arec::psisi::manager>        ((char*)"PsisiMgr_3"          , 100, 3), // group3

		std::make_shared<arec::tuner_service::manager>        ((char*)"TunerService"        , 10),
		std::make_shared<arec::rec::manager>          ((char*)"RecManager"          , 10),
		std::make_shared<arec::channel::manager>      ((char*)"ChannelManager"      , 10),
		std::make_shared<arec::event_schedule::manager>((char*)"EventScheduleManager", 50),
		std::make_shared<arec::event_search::manager>         ((char*)"EventSearch"         , 10),
		std::make_shared<arec::viewing::manager>      ((char*)"ViewingManager"      , 10),
		std::make_shared<arec::command_server::manager>       ((char*)"CommandServer"       , 10),
	};
}


std::vector<std::shared_ptr<coseq::module_base>> &get_modules (void)
{
	build_modules ();
	return s_modules;
}

coseq::module_base *get_module (module_id id)
{
	build_modules ();
	if (id < module_id::max) {
		return s_modules [static_cast<int>(id)].get();
	} else {
		return nullptr;
	}
}


static coseq::manager *sp_manager = nullptr;

void set_manager (coseq::manager *mgr)
{
	sp_manager = mgr;
}

coseq::manager *get_manager (void)
{
	return sp_manager;
}


static std::atomic<bool> s_shutdown_requested {false};

void request_shutdown (void)
{
	s_shutdown_requested.store (true);
}

bool is_shutdown_requested (void)
{
	return s_shutdown_requested.load ();
}

} // namespace modules
