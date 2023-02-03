#ifndef _EVENT_SEARCH_IF_H_
#define _EVENT_SEARCH_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "ThreadMgrpp.h"
#include "modules.h"
#include "threadmgr_if.h"


class CEventSearchIf : public threadmgr::CThreadMgrExternalIf
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

	explicit CEventSearchIf (CThreadMgrExternalIf *p_if)
		: CThreadMgrExternalIf (p_if)
		, m_module_id (static_cast<uint8_t>(module::module_id::event_search))
	{
	};

	virtual ~CEventSearchIf (void) {
	};


	bool request_module_up (void) {
		int sequence = static_cast<int>(sequence::module_up);
		return request_async (m_module_id, sequence);
	};

	bool request_module_down (void) {
		int sequence = static_cast<int>(sequence::module_down);
		return request_async (m_module_id, sequence);
	};

	bool request_add_rec_reserve_keyword_search (void) {
		int sequence = static_cast<int>(sequence::add_rec_reserve_keyword_search);
		return request_async (m_module_id, sequence);
	};

	bool request_add_rec_reserve_keyword_search_ex (void) {
		int sequence = static_cast<int>(sequence::add_rec_reserve_keyword_search_ex);
		return request_async (m_module_id, sequence);
	};

	bool request_dump_histories (void) {
		int sequence = static_cast<int>(sequence::dump_histories);
		return request_async (m_module_id, sequence);
	};

	bool request_dump_histories_ex (void) {
		int sequence = static_cast<int>(sequence::dump_histories_ex);
		return request_async (m_module_id, sequence);
	};

private:
	uint8_t m_module_id;
};

#endif
