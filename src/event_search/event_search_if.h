#ifndef _EVENT_SEARCH_IF_H_
#define _EVENT_SEARCH_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "client_if.h"
#include "modules.h"


namespace arec {
namespace event_search {

class client_iface : public arec::client_iface_base
{
public:
	enum class sequence : int {
		module_up = 0,
		module_down,
		add_rec_reserve_keyword_search,
		add_rec_reserve_keyword_search_ex,
		dump_histories,
		dump_histories_ex,
		max,
	};

	explicit client_iface (coseq::manager *p_mgr)
		: arec::client_iface_base (p_mgr)
		, module_id_ (static_cast<uint8_t>(modules::module_id::event_search))
	{
	};

	virtual ~client_iface (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_sync (module_id_, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (module_id_, sequence);
	};

	bool request_add_rec_reserve_keyword_search (void) {
		int sequence = static_cast<int>(sequence::add_rec_reserve_keyword_search);
		return request_async (module_id_, sequence);
	};

	bool request_add_rec_reserve_keyword_search_ex (void) {
		int sequence = static_cast<int>(sequence::add_rec_reserve_keyword_search_ex);
		return request_async (module_id_, sequence);
	};

	bool request_dump_histories (void) {
		int sequence = static_cast<int>(sequence::dump_histories);
		return request_async (module_id_, sequence);
	};

	bool request_dump_histories_ex (void) {
		int sequence = static_cast<int>(sequence::dump_histories_ex);
		return request_async (module_id_, sequence);
	};

private:
	uint8_t module_id_;
};

} // namespace event_search
} // namespace arec

#endif
