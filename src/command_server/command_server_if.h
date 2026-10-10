#ifndef _COMMAND_SERVER_IF_H_
#define _COMMAND_SERVER_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "client_if.h"
#include "modules.h"


namespace arec {
namespace command_server {

class client_iface : public arec::client_iface_base
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		server_loop,
		max,
	};

public:
	explicit client_iface (coseq::manager *p_mgr)
		: arec::client_iface_base (p_mgr)
		, module_id_ (static_cast<uint8_t>(modules::module_id::command_server))
	{
	};

	virtual ~client_iface (void) {
	};


	// main が完了を待つので同期要求
	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_sync (module_id_, sequence).get_result() == coseq::result::success;
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (module_id_, sequence);
	};

private:
	uint8_t module_id_;
};

} // namespace command_server
} // namespace arec

#endif
